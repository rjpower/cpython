#ifndef Py_INTERNAL_VALUE_OPS_H
#define Py_INTERNAL_VALUE_OPS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_value.h"

/* Borrowed inputs are retained across callback-capable object bridges. */
extern Py_hash_t _PyValue_Hash(PyValue value);
extern int _PyValue_RichCompareBool(PyValueBorrow left, PyValueBorrow right, int op);
#define _PyValue_RichCompareBool(V0, V1, V2) \
    (_PyValue_RichCompareBool)(PyValue_Borrow(V0), PyValue_Borrow(V1), V2)
/* An owning object result, including arbitrary Python comparison results. */
extern PyObject *_PyValue_RichCompareObject(PyValueBorrow left, PyValueBorrow right, int op);
#define _PyValue_RichCompareObject(V0, V1, V2) \
    (_PyValue_RichCompareObject)(PyValue_Borrow(V0), PyValue_Borrow(V1), V2)
extern int _PyValue_WriteRepr(PyUnicodeWriter *writer, PyValueBorrow value);
#define _PyValue_WriteRepr(V0, V1) (_PyValue_WriteRepr)(V0, PyValue_Borrow(V1))

#ifdef __cplusplus
}
#endif
#endif
