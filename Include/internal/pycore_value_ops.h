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
extern int _PyValue_RichCompareBool(PyValue left, PyValue right, int op);
/* An owning object result, including arbitrary Python comparison results. */
extern PyObject *_PyValue_RichCompareObject(PyValue left, PyValue right, int op);
extern int _PyValue_WriteRepr(PyUnicodeWriter *writer, PyValue value);

#ifdef __cplusplus
}
#endif
#endif
