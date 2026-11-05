#ifndef Py_INTERNAL_VALUE_OPS_H
#define Py_INTERNAL_VALUE_OPS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_value.h"

/* The value dispatchers (pyjit/design/proposals/value-api-v2a0.md, section
   4.6). Each is the port of an object protocol function, which is now its
   thunk: it calls a type's value slot, or its object slot through the
   adapter when the value slot is 0, with the same arguments in the same
   order. The operands are borrows that stay valid for the whole call, the
   Python code that a slot runs included: the caller holds the owners. A
   PyValue result is an owner, or PyValue_NULL with an exception set. Each
   has a same-name macro that applies PyValue_Borrow(). Exported for the
   shared in-tree modules (W4). */

#define _PyValue_BINARY_DISPATCHERS(X) \
    X(Add) X(Subtract) X(Multiply) X(Remainder) X(Divmod) X(Lshift) \
    X(Rshift) X(And) X(Xor) X(Or) X(FloorDivide) X(TrueDivide) \
    X(MatrixMultiply)
#define _PyValue_INPLACE_DISPATCHERS(X) \
    X(Add) X(Subtract) X(Multiply) X(Remainder) X(Lshift) X(Rshift) \
    X(And) X(Xor) X(Or) X(FloorDivide) X(TrueDivide) X(MatrixMultiply)

#define _PyValue_DECLARE_BINARY(NAME) \
    PyAPI_FUNC(PyValue) _PyValue_##NAME(PyValueBorrow v, PyValueBorrow w);
#define _PyValue_DECLARE_INPLACE(NAME) \
    PyAPI_FUNC(PyValue) _PyValue_InPlace##NAME(PyValueBorrow v, \
                                               PyValueBorrow w);
_PyValue_BINARY_DISPATCHERS(_PyValue_DECLARE_BINARY)
_PyValue_INPLACE_DISPATCHERS(_PyValue_DECLARE_INPLACE)
#undef _PyValue_DECLARE_BINARY
#undef _PyValue_DECLARE_INPLACE

#define _PyValue_Add(V, W) (_PyValue_Add)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_Subtract(V, W) \
    (_PyValue_Subtract)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_Multiply(V, W) \
    (_PyValue_Multiply)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_Remainder(V, W) \
    (_PyValue_Remainder)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_Divmod(V, W) \
    (_PyValue_Divmod)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_Lshift(V, W) \
    (_PyValue_Lshift)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_Rshift(V, W) \
    (_PyValue_Rshift)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_And(V, W) (_PyValue_And)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_Xor(V, W) (_PyValue_Xor)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_Or(V, W) (_PyValue_Or)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_FloorDivide(V, W) \
    (_PyValue_FloorDivide)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_TrueDivide(V, W) \
    (_PyValue_TrueDivide)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_MatrixMultiply(V, W) \
    (_PyValue_MatrixMultiply)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_InPlaceAdd(V, W) \
    (_PyValue_InPlaceAdd)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_InPlaceSubtract(V, W) \
    (_PyValue_InPlaceSubtract)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_InPlaceMultiply(V, W) \
    (_PyValue_InPlaceMultiply)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_InPlaceRemainder(V, W) \
    (_PyValue_InPlaceRemainder)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_InPlaceLshift(V, W) \
    (_PyValue_InPlaceLshift)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_InPlaceRshift(V, W) \
    (_PyValue_InPlaceRshift)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_InPlaceAnd(V, W) \
    (_PyValue_InPlaceAnd)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_InPlaceXor(V, W) \
    (_PyValue_InPlaceXor)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_InPlaceOr(V, W) \
    (_PyValue_InPlaceOr)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_InPlaceFloorDivide(V, W) \
    (_PyValue_InPlaceFloorDivide)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_InPlaceTrueDivide(V, W) \
    (_PyValue_InPlaceTrueDivide)(PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_InPlaceMatrixMultiply(V, W) \
    (_PyValue_InPlaceMatrixMultiply)(PyValue_Borrow(V), PyValue_Borrow(W))

// v ** w, or pow(v, w, z); z is None for two operands.
PyAPI_FUNC(PyValue) _PyValue_Power(PyValueBorrow v, PyValueBorrow w,
                                   PyValueBorrow z);
PyAPI_FUNC(PyValue) _PyValue_InPlacePower(PyValueBorrow v, PyValueBorrow w,
                                          PyValueBorrow z);
#define _PyValue_Power(V, W, Z) \
    (_PyValue_Power)(PyValue_Borrow(V), PyValue_Borrow(W), PyValue_Borrow(Z))
#define _PyValue_InPlacePower(V, W, Z) \
    (_PyValue_InPlacePower)(PyValue_Borrow(V), PyValue_Borrow(W), \
                            PyValue_Borrow(Z))

PyAPI_FUNC(PyValue) _PyValue_Negative(PyValueBorrow v);
PyAPI_FUNC(PyValue) _PyValue_Positive(PyValueBorrow v);
PyAPI_FUNC(PyValue) _PyValue_Absolute(PyValueBorrow v);
PyAPI_FUNC(PyValue) _PyValue_Invert(PyValueBorrow v);
#define _PyValue_Negative(V) (_PyValue_Negative)(PyValue_Borrow(V))
#define _PyValue_Positive(V) (_PyValue_Positive)(PyValue_Borrow(V))
#define _PyValue_Absolute(V) (_PyValue_Absolute)(PyValue_Borrow(V))
#define _PyValue_Invert(V) (_PyValue_Invert)(PyValue_Borrow(V))

/* _PyNumber_Index(): an int, possibly of a subclass; PyNumber_Long() and
   PyNumber_Float(); PyNumber_AsSsize_t(), which reads an inline int
   directly. */
PyAPI_FUNC(PyValue) _PyValue_Index(PyValueBorrow v);
PyAPI_FUNC(PyValue) _PyValue_Long(PyValueBorrow v);
PyAPI_FUNC(PyValue) _PyValue_Float(PyValueBorrow v);
PyAPI_FUNC(Py_ssize_t) _PyValue_AsSsize_t(PyValueBorrow v, PyObject *exc);
#define _PyValue_Index(V) (_PyValue_Index)(PyValue_Borrow(V))
#define _PyValue_Long(V) (_PyValue_Long)(PyValue_Borrow(V))
#define _PyValue_Float(V) (_PyValue_Float)(PyValue_Borrow(V))
#define _PyValue_AsSsize_t(V, E) (_PyValue_AsSsize_t)(PyValue_Borrow(V), E)

/* PyObject_RichCompare() and PyObject_RichCompareBool(); the latter keeps
   value_ops.c's inline arm ahead of the dispatch and its owners of the
   operands, which its callers in the containers rely on. */
PyAPI_FUNC(PyValue) _PyValue_RichCompare(PyValueBorrow v, PyValueBorrow w,
                                         int op);
PyAPI_FUNC(int) _PyValue_RichCompareBool(PyValueBorrow v, PyValueBorrow w,
                                         int op);
#define _PyValue_RichCompare(V, W, OP) \
    (_PyValue_RichCompare)(PyValue_Borrow(V), PyValue_Borrow(W), OP)
#define _PyValue_RichCompareBool(V, W, OP) \
    (_PyValue_RichCompareBool)(PyValue_Borrow(V), PyValue_Borrow(W), OP)

PyAPI_FUNC(int) _PyValue_IsTrue(PyValueBorrow v);
PyAPI_FUNC(int) _PyValue_Not(PyValueBorrow v);
#define _PyValue_IsTrue(V) (_PyValue_IsTrue)(PyValue_Borrow(V))
#define _PyValue_Not(V) (_PyValue_Not)(PyValue_Borrow(V))

/* PyObject_Hash(), after value_ops.c's inline arms, with an owner of the
   operand in case __hash__ removes it from the container that it was read
   from. */
PyAPI_FUNC(Py_hash_t) _PyValue_Hash(PyValueBorrow v);
#define _PyValue_Hash(V) (_PyValue_Hash)(PyValue_Borrow(V))

PyAPI_FUNC(PyObject *) _PyValue_Repr(PyValueBorrow v);
PyAPI_FUNC(PyObject *) _PyValue_Str(PyValueBorrow v);
PyAPI_FUNC(PyObject *) _PyValue_ASCII(PyValueBorrow v);
#define _PyValue_Repr(V) (_PyValue_Repr)(PyValue_Borrow(V))
#define _PyValue_Str(V) (_PyValue_Str)(PyValue_Borrow(V))
#define _PyValue_ASCII(V) (_PyValue_ASCII)(PyValue_Borrow(V))

/* Mixed paths of the containers. Borrowed inputs are retained across
   callback-capable object bridges. */
/* An owning object result, including arbitrary Python comparison results. */
extern PyObject *_PyValue_RichCompareObject(PyValueBorrow left, PyValueBorrow right, int op);
#define _PyValue_RichCompareObject(V0, V1, V2) \
    (_PyValue_RichCompareObject)(PyValue_Borrow(V0), PyValue_Borrow(V1), V2)
extern int _PyValue_WriteRepr(PyUnicodeWriter *writer, PyValueBorrow value);
#define _PyValue_WriteRepr(V0, V1) (_PyValue_WriteRepr)(V0, PyValue_Borrow(V1))

/* The value sides of the pair table's hash functions
   (pyjit/design/proposals/value-api-v2a0.md, section 4.3):
   PyObject_HashNotImplemented() and PyObject_GenericHash(). */
PyAPI_FUNC(Py_hash_t) _PyValue_HashNotImplemented(PyValueBorrow value);
PyAPI_FUNC(Py_hash_t) _PyValue_GenericHash(PyValueBorrow value);

#ifdef __cplusplus
}
#endif
#endif
