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
   order. A PyValue result is an owner, or PyValue_NULL with an exception
   set. Each has a same-name macro that applies PyValue_Borrow(). Exported
   for the shared in-tree modules (W4).

   The dispatcher contract: the operands are borrows that stay valid for the
   whole call, the Python code that a slot runs included, because the
   caller holds an owner of each. A caller that read an operand from a
   container takes its own owner first (PyValue_DUP), as upstream's callers
   Py_INCREF() an item before PyObject_RichCompare(); the dispatchers take
   none. */

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

/* The binary dispatchers by BINARY_OP's oparg (NB_ADD ... NB_INPLACE_XOR;
   NB_POWER and NB_INPLACE_POWER with a None modulus), for I1's generic
   BINARY_OP handler; NB_SUBSCR is 0 (D14). */
typedef PyValue (*_PyValue_binaryop)(PyValueBorrow, PyValueBorrow);
PyAPI_DATA(const _PyValue_binaryop) _PyValue_BinaryOps[];

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

/* PyObject_RichCompare() and PyObject_RichCompareBool(); the latter
   compares two inline values by R-inline (value_ops.c). */
PyAPI_FUNC(PyValue) _PyValue_RichCompare(PyValueBorrow v, PyValueBorrow w,
                                         int op);
PyAPI_FUNC(int) _PyValue_RichCompareBool(PyValueBorrow v, PyValueBorrow w,
                                         int op);
#define _PyValue_RichCompare(V, W, OP) \
    (_PyValue_RichCompare)(PyValue_Borrow(V), PyValue_Borrow(W), OP)
#define _PyValue_RichCompareBool(V, W, OP) \
    (_PyValue_RichCompareBool)(PyValue_Borrow(V), PyValue_Borrow(W), OP)

/* The compare and repr pairs of type t (object.c): the value slot of the
   row if it is set, else the object slot through the adapter. object's own
   value sides call them for the receiver's type (typeobject.c). */
extern PyValue _PyValue_CallRichCompare(PyTypeObject *t, richcmpfunc slot,
                                        PyValueBorrow v, PyValueBorrow w,
                                        int op);
extern PyObject *_PyValue_CallRepr(PyTypeObject *t, reprfunc slot, int row,
                                   PyValueBorrow v);

/* object's value sides (typeobject.c), which NoneType and the sentinel type
   inherit: upstream's object_richcompare() (== is identity, else
   NotImplemented; != inverts the receiver type's ==) and object_str() (the
   type's repr). Their object sides are _Py_BaseObject_RichCompare() and
   object_str(). */
PyAPI_FUNC(PyValue) _PyObject_ValueRichCompare(PyValueBorrow v,
                                               PyValueBorrow w, int op);
PyAPI_FUNC(PyObject *) _PyObject_ValueStr(PyValueBorrow v);

PyAPI_FUNC(int) _PyValue_IsTrue(PyValueBorrow v);
PyAPI_FUNC(int) _PyValue_Not(PyValueBorrow v);
#define _PyValue_IsTrue(V) (_PyValue_IsTrue)(PyValue_Borrow(V))
#define _PyValue_Not(V) (_PyValue_Not)(PyValue_Borrow(V))

/* PyObject_Hash(); an inline value is hashed by R-inline (value_ops.c). */
PyAPI_FUNC(Py_hash_t) _PyValue_Hash(PyValueBorrow v);
#define _PyValue_Hash(V) (_PyValue_Hash)(PyValue_Borrow(V))

PyAPI_FUNC(PyObject *) _PyValue_Repr(PyValueBorrow v);
PyAPI_FUNC(PyObject *) _PyValue_Str(PyValueBorrow v);
PyAPI_FUNC(PyObject *) _PyValue_ASCII(PyValueBorrow v);
#define _PyValue_Repr(V) (_PyValue_Repr)(PyValue_Borrow(V))
#define _PyValue_Str(V) (_PyValue_Str)(PyValue_Borrow(V))
#define _PyValue_ASCII(V) (_PyValue_ASCII)(PyValue_Borrow(V))

/* Attribute and descriptor dispatch (pyjit/design/proposals/
   value-api-m1b-m2.md, section 2.2). The receiver v, and a descriptor's
   obj, may be inline; name must be a str (a TypeError otherwise). A
   _PyStatusValue result has status 1 (found, value owned), 0 (missing:
   value NULL, no exception) or -1 (error). */
PyAPI_FUNC(PyValue) _PyValue_GetAttr(PyValueBorrow v, PyObject *name);
PyAPI_FUNC(_PyStatusValue) _PyValue_GetOptionalAttr(PyValueBorrow v,
                                                    PyObject *name);
/* A PyValue_NULL value deletes. */
PyAPI_FUNC(int) _PyValue_SetAttr(PyValueBorrow v, PyObject *name,
                                 PyValueBorrow value);
/* Status 1: an unbound method, to call with obj as its first argument;
   0: the attribute itself. */
PyAPI_FUNC(_PyStatusValue) _PyValue_GetMethod(PyValueBorrow obj,
                                              PyObject *name);
/* The tp_descr_get and tp_descr_set of descr's type, through its value
   slot or the adapter. obj is PyValue_NULL for class access; type may be
   NULL. */
PyAPI_FUNC(PyValue) _PyValue_DescrGet(PyObject *descr, PyValueBorrow obj,
                                      PyTypeObject *type);
PyAPI_FUNC(int) _PyValue_DescrSet(PyObject *descr, PyValueBorrow obj,
                                  PyValueBorrow value);
/* The special method name of self's type, bound to self by its
   descriptor: _PyObject_LookupSpecial(). */
PyAPI_FUNC(_PyStatusValue) _PyValue_LookupSpecial(PyValueBorrow self,
                                                  PyObject *name);
#define _PyValue_GetAttr(V, N) (_PyValue_GetAttr)(PyValue_Borrow(V), N)
#define _PyValue_GetOptionalAttr(V, N) \
    (_PyValue_GetOptionalAttr)(PyValue_Borrow(V), N)
#define _PyValue_SetAttr(V, N, W) \
    (_PyValue_SetAttr)(PyValue_Borrow(V), N, PyValue_Borrow(W))
#define _PyValue_GetMethod(V, N) (_PyValue_GetMethod)(PyValue_Borrow(V), N)
#define _PyValue_DescrGet(D, V, T) (_PyValue_DescrGet)(D, PyValue_Borrow(V), T)
#define _PyValue_DescrSet(D, V, W) \
    (_PyValue_DescrSet)(D, PyValue_Borrow(V), PyValue_Borrow(W))
#define _PyValue_LookupSpecial(V, N) \
    (_PyValue_LookupSpecial)(PyValue_Borrow(V), N)

/* The value sides of the generic attribute pair (section 2.4), which the
   pair table gives every type whose object slots are
   PyObject_GenericGetAttr and PyObject_GenericSetAttr. A PyValue_NULL value
   deletes. _PyObject_GenericLookupAttrValue() reads dict, if not NULL, as
   the instance dict; a missing attribute gives status 0 without an
   exception. */
PyAPI_FUNC(PyValue) _PyObject_GenericGetAttrValue(PyValueBorrow v,
                                                  PyObject *name);
PyAPI_FUNC(_PyStatusValue) _PyObject_GenericLookupAttrValue(
    PyValueBorrow v, PyObject *name, PyObject *dict);
PyAPI_FUNC(int) _PyObject_GenericSetAttrValue(PyValueBorrow v, PyObject *name,
                                              PyValueBorrow value);
PyAPI_FUNC(int) _PyObject_GenericSetAttrWithDictValue(
    PyValueBorrow v, PyObject *name, PyValueBorrow value, PyObject *dict);
#define _PyObject_GenericGetAttrValue(V, N) \
    (_PyObject_GenericGetAttrValue)(PyValue_Borrow(V), N)
#define _PyObject_GenericLookupAttrValue(V, N, D) \
    (_PyObject_GenericLookupAttrValue)(PyValue_Borrow(V), N, D)
#define _PyObject_GenericSetAttrValue(V, N, W) \
    (_PyObject_GenericSetAttrValue)(PyValue_Borrow(V), N, PyValue_Borrow(W))
#define _PyObject_GenericSetAttrWithDictValue(V, N, W, D) \
    (_PyObject_GenericSetAttrWithDictValue)(PyValue_Borrow(V), N, \
                                            PyValue_Borrow(W), D)

/* name in the MRO of type: status 1 with an owner, or 0; it never fails
   (_PyType_LookupRef()). */
extern _PyStatusValue _PyType_LookupValue(PyTypeObject *type, PyObject *name);

/* The value form of _PySuper_Lookup(): status 1 is an unbound method, to
   call with su_obj (only if want_method), 0 the attribute. */
extern _PyStatusValue _PySuper_LookupValue(PyTypeObject *su_type,
                                           PyValueBorrow su_obj,
                                           PyObject *name, int want_method);
#define _PySuper_LookupValue(T, V, N, M) \
    (_PySuper_LookupValue)(T, PyValue_Borrow(V), N, M)

/* Paths of the containers. An owning object result, including arbitrary
   Python comparison results. */
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

/* Subscript, item, contains, size, iteration and send
   (pyjit/design/proposals/value-api-m1b-m2.md, section 1.2). A receiver
   that is a value is tested for an inline value before any slot is read:
   no int or float defines these slots, so an inline receiver gets
   upstream's TypeError. An iterator is an object, the result of
   _PyValue_GetIter(). A _PyStatusValue result has status -1 (error,
   PyValue_NULL), 0 (absent, ended or returned; the value, if any, is
   owned) or 1 (found or next; the value is owned). */

// PyObject_GetItem(), PyObject_SetItem() and PyObject_DelItem().
PyAPI_FUNC(PyValue) _PyValue_GetItem(PyValueBorrow o, PyValueBorrow key);
PyAPI_FUNC(int) _PyValue_SetItem(PyValueBorrow o, PyValueBorrow key,
                                 PyValueBorrow value);
PyAPI_FUNC(int) _PyValue_DelItem(PyValueBorrow o, PyValueBorrow key);
#define _PyValue_GetItem(O, K) \
    (_PyValue_GetItem)(PyValue_Borrow(O), PyValue_Borrow(K))
#define _PyValue_SetItem(O, K, V) \
    (_PyValue_SetItem)(PyValue_Borrow(O), PyValue_Borrow(K), PyValue_Borrow(V))
#define _PyValue_DelItem(O, K) \
    (_PyValue_DelItem)(PyValue_Borrow(O), PyValue_Borrow(K))

/* PySequence_GetItem(), PySequence_SetItem() and PySequence_DelItem(): a
   negative index is adjusted by the object sq_length. */
PyAPI_FUNC(PyValue) _PyValue_SequenceGetItem(PyValueBorrow s, Py_ssize_t i);
PyAPI_FUNC(int) _PyValue_SequenceSetItem(PyValueBorrow s, Py_ssize_t i,
                                         PyValueBorrow value);
PyAPI_FUNC(int) _PyValue_SequenceDelItem(PyValueBorrow s, Py_ssize_t i);
#define _PyValue_SequenceGetItem(S, I) \
    (_PyValue_SequenceGetItem)(PyValue_Borrow(S), I)
#define _PyValue_SequenceSetItem(S, I, V) \
    (_PyValue_SequenceSetItem)(PyValue_Borrow(S), I, PyValue_Borrow(V))
#define _PyValue_SequenceDelItem(S, I) \
    (_PyValue_SequenceDelItem)(PyValue_Borrow(S), I)

/* PyMapping_GetOptionalItem(): 1 with the item, 0 for a missing key (a
   KeyError is cleared), -1 on another error. */
PyAPI_FUNC(_PyStatusValue) _PyValue_GetOptionalItem(PyValueBorrow o,
                                                    PyValueBorrow key);
#define _PyValue_GetOptionalItem(O, K) \
    (_PyValue_GetOptionalItem)(PyValue_Borrow(O), PyValue_Borrow(K))

// PySequence_Contains() and _PySequence_IterSearch().
PyAPI_FUNC(int) _PyValue_Contains(PyValueBorrow seq, PyValueBorrow item);
PyAPI_FUNC(Py_ssize_t) _PyValue_IterSearch(PyValueBorrow seq,
                                           PyValueBorrow obj, int operation);
#define _PyValue_Contains(S, I) \
    (_PyValue_Contains)(PyValue_Borrow(S), PyValue_Borrow(I))
#define _PyValue_IterSearch(S, O, OP) \
    (_PyValue_IterSearch)(PyValue_Borrow(S), PyValue_Borrow(O), OP)

// PyObject_Size() and PyObject_GetIter().
PyAPI_FUNC(Py_ssize_t) _PyValue_Size(PyValueBorrow o);
PyAPI_FUNC(PyObject *) _PyValue_GetIter(PyValueBorrow o);
#define _PyValue_Size(O) (_PyValue_Size)(PyValue_Borrow(O))
#define _PyValue_GetIter(O) (_PyValue_GetIter)(PyValue_Borrow(O))

/* The three iteration layers (section 1.2; R11, R16).
   _PyValue_IterStep(): one call of the iterator's value iternext slot, or
   of its object tp_iternext through the adapter, unnormalised: 1 with the
   item; 0 with the return value, PyValue_NULL for none; -1 with any
   exception, StopIteration included, untouched. For next(), the wrapping
   iterators and FOR_ITER.
   _PyValue_IterNext(): the consumer's step (PyIter_Next()): 1 with the
   item, 0 at the end (a return value is closed, a pending StopIteration
   cleared), -1 on another error.
   _PyValue_Send(): PyIter_Send(): 1 with the yielded item, 0 with the
   return value (None if there is none), -1 on error. */
PyAPI_FUNC(_PyStatusValue) _PyValue_IterStep(PyObject *it);
PyAPI_FUNC(_PyStatusValue) _PyValue_IterNext(PyObject *it);
PyAPI_FUNC(_PyStatusValue) _PyValue_Send(PyObject *it, PyValueBorrow arg);
#define _PyValue_Send(IT, A) (_PyValue_Send)(IT, PyValue_Borrow(A))

/* The value side of _PyObject_NextNotImplemented() in the pair table: the
   iternext of a type that is not an iterator. */
PyAPI_FUNC(_PyStatusValue) _PyValue_NextNotImplemented(PyObject *it);

/* The thin value forms of the rest of the sequence, mapping and iteration
   families follow. */

// PyObject_LengthHint() and _PyObject_HasLen().
PyAPI_FUNC(Py_ssize_t) _PyValue_LengthHint(PyValueBorrow v,
                                           Py_ssize_t defaultvalue);
PyAPI_FUNC(int) _PyValue_HasLen(PyValueBorrow v);
#define _PyValue_LengthHint(V, DEFAULTVALUE) \
    (_PyValue_LengthHint)(PyValue_Borrow(V), DEFAULTVALUE)
#define _PyValue_HasLen(V) \
    (_PyValue_HasLen)(PyValue_Borrow(V))

// PyObject_DelItemString().
PyAPI_FUNC(int) _PyValue_DelItemString(PyValueBorrow o, const char *key);
#define _PyValue_DelItemString(O, KEY) \
    (_PyValue_DelItemString)(PyValue_Borrow(O), KEY)

// PySequence_Check(), PySequence_Size() and PySequence_Length().
PyAPI_FUNC(int) _PyValue_SequenceCheck(PyValueBorrow s);
PyAPI_FUNC(Py_ssize_t) _PyValue_SequenceSize(PyValueBorrow s);
#define _PyValue_SequenceCheck(S) \
    (_PyValue_SequenceCheck)(PyValue_Borrow(S))
#define _PyValue_SequenceSize(S) \
    (_PyValue_SequenceSize)(PyValue_Borrow(S))

// PySequence_Concat(), _Repeat(), _InPlaceConcat() and _InPlaceRepeat().
PyAPI_FUNC(PyValue) _PyValue_SequenceConcat(PyValueBorrow s, PyValueBorrow o);
PyAPI_FUNC(PyValue) _PyValue_SequenceRepeat(PyValueBorrow o, Py_ssize_t count);
PyAPI_FUNC(PyValue) _PyValue_SequenceInPlaceConcat(PyValueBorrow s,
                                                   PyValueBorrow o);
PyAPI_FUNC(PyValue) _PyValue_SequenceInPlaceRepeat(PyValueBorrow o,
                                                   Py_ssize_t count);
#define _PyValue_SequenceConcat(S, O) \
    (_PyValue_SequenceConcat)(PyValue_Borrow(S), PyValue_Borrow(O))
#define _PyValue_SequenceRepeat(O, COUNT) \
    (_PyValue_SequenceRepeat)(PyValue_Borrow(O), COUNT)
#define _PyValue_SequenceInPlaceConcat(S, O) \
    (_PyValue_SequenceInPlaceConcat)(PyValue_Borrow(S), PyValue_Borrow(O))
#define _PyValue_SequenceInPlaceRepeat(O, COUNT) \
    (_PyValue_SequenceInPlaceRepeat)(PyValue_Borrow(O), COUNT)

// PySequence_GetSlice(), _SetSlice() (a null item deletes) and _DelSlice().
PyAPI_FUNC(PyValue) _PyValue_SequenceGetSlice(PyValueBorrow s, Py_ssize_t i1,
                                              Py_ssize_t i2);
PyAPI_FUNC(int) _PyValue_SequenceSetSlice(PyValueBorrow s, Py_ssize_t i1,
                                          Py_ssize_t i2, PyValueBorrow o);
PyAPI_FUNC(int) _PyValue_SequenceDelSlice(PyValueBorrow s, Py_ssize_t i1,
                                          Py_ssize_t i2);
#define _PyValue_SequenceGetSlice(S, I1, I2) \
    (_PyValue_SequenceGetSlice)(PyValue_Borrow(S), I1, I2)
#define _PyValue_SequenceSetSlice(S, I1, I2, O) \
    (_PyValue_SequenceSetSlice)(PyValue_Borrow(S), I1, I2, PyValue_Borrow(O))
#define _PyValue_SequenceDelSlice(S, I1, I2) \
    (_PyValue_SequenceDelSlice)(PyValue_Borrow(S), I1, I2)

// PySequence_Tuple(), PySequence_List() and PySequence_Fast().
PyAPI_FUNC(PyObject *) _PyValue_SequenceTuple(PyValueBorrow v);
PyAPI_FUNC(PyObject *) _PyValue_SequenceList(PyValueBorrow v);
PyAPI_FUNC(PyObject *) _PyValue_SequenceFast(PyValueBorrow v, const char *m);
#define _PyValue_SequenceTuple(V) \
    (_PyValue_SequenceTuple)(PyValue_Borrow(V))
#define _PyValue_SequenceList(V) \
    (_PyValue_SequenceList)(PyValue_Borrow(V))
#define _PyValue_SequenceFast(V, M) \
    (_PyValue_SequenceFast)(PyValue_Borrow(V), M)

// PySequence_Count() and PySequence_Index().
PyAPI_FUNC(Py_ssize_t) _PyValue_SequenceCount(PyValueBorrow s,
                                              PyValueBorrow o);
PyAPI_FUNC(Py_ssize_t) _PyValue_SequenceIndex(PyValueBorrow s,
                                              PyValueBorrow o);
#define _PyValue_SequenceCount(S, O) \
    (_PyValue_SequenceCount)(PyValue_Borrow(S), PyValue_Borrow(O))
#define _PyValue_SequenceIndex(S, O) \
    (_PyValue_SequenceIndex)(PyValue_Borrow(S), PyValue_Borrow(O))

// PyMapping_Check(), PyMapping_Size() and PyMapping_Length().
PyAPI_FUNC(int) _PyValue_MappingCheck(PyValueBorrow o);
PyAPI_FUNC(Py_ssize_t) _PyValue_MappingSize(PyValueBorrow o);
#define _PyValue_MappingCheck(O) \
    (_PyValue_MappingCheck)(PyValue_Borrow(O))
#define _PyValue_MappingSize(O) \
    (_PyValue_MappingSize)(PyValue_Borrow(O))

// PyMapping_GetItemString(), _GetOptionalItemString() and _SetItemString().
PyAPI_FUNC(PyValue) _PyValue_MappingGetItemString(PyValueBorrow o,
                                                  const char *key);
PyAPI_FUNC(_PyStatusValue)
_PyValue_MappingGetOptionalItemString(PyValueBorrow obj, const char *key);
PyAPI_FUNC(int) _PyValue_MappingSetItemString(PyValueBorrow o, const char *key,
                                              PyValueBorrow value);
#define _PyValue_MappingGetItemString(O, KEY) \
    (_PyValue_MappingGetItemString)(PyValue_Borrow(O), KEY)
#define _PyValue_MappingGetOptionalItemString(OBJ, KEY) \
    (_PyValue_MappingGetOptionalItemString)(PyValue_Borrow(OBJ), KEY)
#define _PyValue_MappingSetItemString(O, KEY, VALUE) \
    (_PyValue_MappingSetItemString)(PyValue_Borrow(O), KEY, \
                                    PyValue_Borrow(VALUE))

// PyMapping_HasKey*(). The two that ignore errors (HasKey, HasKeyString)
// also take PyValue_NULL, for the null argument of the object API.
PyAPI_FUNC(int) _PyValue_MappingHasKeyWithError(PyValueBorrow obj,
                                                PyValueBorrow key);
PyAPI_FUNC(int) _PyValue_MappingHasKeyStringWithError(PyValueBorrow obj,
                                                      const char *key);
PyAPI_FUNC(int) _PyValue_MappingHasKey(PyValueBorrow obj, PyValueBorrow key);
PyAPI_FUNC(int) _PyValue_MappingHasKeyString(PyValueBorrow obj,
                                             const char *key);
#define _PyValue_MappingHasKeyWithError(OBJ, KEY) \
    (_PyValue_MappingHasKeyWithError)(PyValue_Borrow(OBJ), PyValue_Borrow(KEY))
#define _PyValue_MappingHasKeyStringWithError(OBJ, KEY) \
    (_PyValue_MappingHasKeyStringWithError)(PyValue_Borrow(OBJ), KEY)
#define _PyValue_MappingHasKey(OBJ, KEY) \
    (_PyValue_MappingHasKey)(PyValue_Borrow(OBJ), PyValue_Borrow(KEY))
#define _PyValue_MappingHasKeyString(OBJ, KEY) \
    (_PyValue_MappingHasKeyString)(PyValue_Borrow(OBJ), KEY)

// PyMapping_Keys(), _Items() and _Values().
PyAPI_FUNC(PyObject *) _PyValue_MappingKeys(PyValueBorrow o);
PyAPI_FUNC(PyObject *) _PyValue_MappingItems(PyValueBorrow o);
PyAPI_FUNC(PyObject *) _PyValue_MappingValues(PyValueBorrow o);
#define _PyValue_MappingKeys(O) \
    (_PyValue_MappingKeys)(PyValue_Borrow(O))
#define _PyValue_MappingItems(O) \
    (_PyValue_MappingItems)(PyValue_Borrow(O))
#define _PyValue_MappingValues(O) \
    (_PyValue_MappingValues)(PyValue_Borrow(O))

#ifdef __cplusplus
}
#endif
#endif
