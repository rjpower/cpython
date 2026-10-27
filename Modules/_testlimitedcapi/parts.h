#ifndef Py_TESTLIMITEDCAPI_PARTS_H
#define Py_TESTLIMITEDCAPI_PARTS_H

// Always enable assertions
#undef NDEBUG


// Argument Clinic's output uses the internal C API.
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

int _PyTestLimitedCAPI_Init_Abstract(PyObject *module);
int _PyTestLimitedCAPI_Init_ByteArray(PyObject *module);
int _PyTestLimitedCAPI_Init_Bytes(PyObject *module);
int _PyTestLimitedCAPI_Init_Capsule(PyObject *module);
int _PyTestLimitedCAPI_Init_Codec(PyObject *module);
int _PyTestLimitedCAPI_Init_Complex(PyObject *module);
int _PyTestLimitedCAPI_Init_Dict(PyObject *module);
int _PyTestLimitedCAPI_Init_Eval(PyObject *module);
int _PyTestLimitedCAPI_Init_Float(PyObject *module);
int _PyTestLimitedCAPI_Init_HeaptypeRelative(PyObject *module);
int _PyTestLimitedCAPI_Init_Import(PyObject *module);
int _PyTestLimitedCAPI_Init_Object(PyObject *module);
int _PyTestLimitedCAPI_Init_List(PyObject *module);
int _PyTestLimitedCAPI_Init_Long(PyObject *module);
int _PyTestLimitedCAPI_Init_PyOS(PyObject *module);
int _PyTestLimitedCAPI_Init_Set(PyObject *module);
int _PyTestLimitedCAPI_Init_Slice(PyObject *module);
int _PyTestLimitedCAPI_Init_Slots(PyObject *module);
int _PyTestLimitedCAPI_Init_Sys(PyObject *module);
int _PyTestLimitedCAPI_Init_ThreadState(PyObject *module);
int _PyTestLimitedCAPI_Init_Tuple(PyObject *module);
int _PyTestLimitedCAPI_Init_Unicode(PyObject *module);
int _PyTestLimitedCAPI_Init_VectorcallLimited(PyObject *module);
int _PyTestLimitedCAPI_Init_Version(PyObject *module);
int _PyTestLimitedCAPI_Init_File(PyObject *module);
int _PyTestLimitedCAPI_Init_Weakref(PyObject *module);
int _PyTestLimitedCAPI_Init_Run(PyObject *module);
int _PyTestLimitedCAPI_Init_Type(PyObject *module);
int _PyTestLimitedCAPI_Init_Hash(PyObject *module);
int _PyTestLimitedCAPI_Init_Build(PyObject *module);

#endif // Py_TESTLIMITEDCAPI_PARTS_H
