#ifndef Py_CPYTHON_ABSTRACTOBJECT_H
#  error "this header file must not be included directly"
#endif

/* === Object Protocol ================================================== */

/* Like PyObject_CallMethod(), but expect a _Py_Identifier*
   as the method name. */
Py_DEPRECATED(3.15) PyAPI_FUNC(PyObject*) _PyObject_CallMethodId(
    PyObject *obj,
    _Py_Identifier *name,
    const char *format, ...);

/* Convert keyword arguments from the FASTCALL (stack: C array, kwnames: tuple)
   format to a Python dictionary ("kwargs" dict).

   The type of kwnames keys is not checked. The final function getting
   arguments is responsible to check if all keys are strings, for example using
   PyArg_ParseTupleAndKeywords() or PyArg_ValidateKeywordArguments().

   Duplicate keys are merged using the last value. If duplicate keys must raise
   an exception, the caller is responsible to implement an explicit keys on
   kwnames. */
PyAPI_FUNC(PyObject*) _PyStack_AsDict(const PyValue *values, PyObject *kwnames);


/* === Vectorcall protocol (PEP 590) ============================= */

// PyVectorcall_NARGS() is exported as a function for the stable ABI.
// Here (when we are not using the stable ABI), the name is overridden to
// call a static inline function for best performance.
static inline Py_ssize_t
_PyVectorcall_NARGS(size_t n)
{
    return n & ~PY_VECTORCALL_ARGUMENTS_OFFSET;
}
#define PyVectorcall_NARGS(n) _PyVectorcall_NARGS(n)

PyAPI_FUNC(vectorcallfunc) PyVectorcall_Function(PyObject *callable);

/* Call callable with the vectorcall convention on values: args holds
   PyVectorcall_NARGS(nargsf) positional arguments and then the values of the
   keywords named in kwnames (a tuple of str, or NULL).  Return the result,
   or PyValue_NULL with an exception set. */
PyAPI_FUNC(PyValue) PyObject_VectorcallValue(
    PyValue callable,
    const PyValue *args,
    size_t nargsf,
    PyObject *kwnames);

// Backwards compatibility aliases (PEP 590) for API that was provisional
// in Python 3.8
#define _PyObject_Vectorcall PyObject_Vectorcall
#define _PyObject_VectorcallMethod PyObject_VectorcallMethod
#define _PyObject_FastCallDict PyObject_VectorcallDict
#define _PyVectorcall_Function PyVectorcall_Function
#define _PyObject_CallOneArg PyObject_CallOneArg
#define _PyObject_CallMethodNoArgs PyObject_CallMethodNoArgs
#define _PyObject_CallMethodOneArg PyObject_CallMethodOneArg

/* Same as PyObject_Vectorcall except that keyword arguments are passed as
   dict, which may be NULL if there are no keyword arguments. */
PyAPI_FUNC(PyObject *) PyObject_VectorcallDict(
    PyObject *callable,
    PyObject *const *args,
    size_t nargsf,
    PyObject *kwargs);

PyAPI_FUNC(PyObject *) PyObject_CallOneArg(PyObject *func, PyObject *arg);

static inline PyObject *
PyObject_CallMethodNoArgs(PyObject *self, PyObject *name)
{
    size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    return PyObject_VectorcallMethod(name, &self, nargsf, _Py_NULL);
}

static inline PyObject *
PyObject_CallMethodOneArg(PyObject *self, PyObject *name, PyObject *arg)
{
    PyObject *args[2] = {self, arg};
    size_t nargsf = 2 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    assert(arg != NULL);
    return PyObject_VectorcallMethod(name, args, nargsf, _Py_NULL);
}

/* Guess the size of object 'o' using len(o) or o.__length_hint__().
   If neither of those return a non-negative value, then return the default
   value.  If one of the calls fails, this function returns -1. */
PyAPI_FUNC(Py_ssize_t) PyObject_LengthHint(PyObject *o, Py_ssize_t);

/* === Sequence protocol ================================================ */

/* Assume tp_as_sequence and sq_item exist and that 'i' does not
   need to be corrected for a negative index. */
#define PySequence_ITEM(o, i)\
    ( Py_TYPE(o)->tp_as_sequence->sq_item((o), (i)) )

/* Return the size of the sequence 'o', assuming that 'o' was returned by
   PySequence_Fast and is not NULL. */
#define PySequence_Fast_GET_SIZE(o) \
    (PyList_Check(o) ? PyList_GET_SIZE(o) : PyTuple_GET_SIZE(o))

/* Return the 'i'-th element of the sequence 'o', assuming that o was returned
   by PySequence_Fast, and that i is within bounds.  It boxes an inline int
   or float, and cannot report an error, so it aborts if boxing fails. */
#define PySequence_Fast_GET_ITEM(o, i)\
     _PyValue_ObjectOrAbort(PySequence_Fast_GET_VALUE((o), (i)), \
                            "PySequence_Fast_GET_ITEM")

/* Return a pointer to the underlying item array, of PyValue, for
   an object returned by PySequence_Fast */
#define PySequence_Fast_ITEMS(sf) \
    (PyList_Check(sf) ? ((PyListObject *)(sf))->ob_item \
                      : ((PyTupleObject *)(sf))->ob_item)

/* The value of the 'i'-th element, under the same conditions as
   PySequence_Fast_GET_ITEM(), which boxes it. */
#define PySequence_Fast_GET_VALUE(o, i)\
     (PyList_Check(o) ? PyList_GET_VALUE((o), (i)) : PyTuple_GET_VALUE((o), (i)))


/* === The PyValue protocols ===========================================

   The protocols on values (Include/pyvalue.h): each takes and returns
   PyValue where its PyObject * counterpart takes and returns PyObject *,
   and means the same.  An inline int or float is worked on without a box
   where the operation is int's or float's own; otherwise it is boxed for
   the type's slot, and a result is always canonical.  A failure returns
   PyValue_NULL, or -1, with an exception set.  The PyObject * functions are
   the compatibility layer over these. */

PyAPI_FUNC(PyValue) PyNumber_AddValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_SubtractValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_MultiplyValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_MatrixMultiplyValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_FloorDivideValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_TrueDivideValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_RemainderValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_PowerValue(PyValue o1, PyValue o2, PyValue o3);
PyAPI_FUNC(PyValue) PyNumber_LshiftValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_RshiftValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_AndValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_OrValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_XorValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_InPlaceAddValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_InPlaceSubtractValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_InPlaceMultiplyValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_InPlaceMatrixMultiplyValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_InPlaceFloorDivideValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_InPlaceTrueDivideValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_InPlaceRemainderValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_InPlacePowerValue(PyValue o1, PyValue o2, PyValue o3);
PyAPI_FUNC(PyValue) PyNumber_InPlaceLshiftValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_InPlaceRshiftValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_InPlaceAndValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_InPlaceOrValue(PyValue o1, PyValue o2);
PyAPI_FUNC(PyValue) PyNumber_InPlaceXorValue(PyValue o1, PyValue o2);

PyAPI_FUNC(PyValue) PyNumber_NegativeValue(PyValue o);
PyAPI_FUNC(PyValue) PyNumber_PositiveValue(PyValue o);
PyAPI_FUNC(PyValue) PyNumber_InvertValue(PyValue o);
PyAPI_FUNC(PyValue) PyNumber_AbsoluteValue(PyValue o);
PyAPI_FUNC(PyValue) PyNumber_IndexValue(PyValue o);
PyAPI_FUNC(PyValue) PyNumber_LongValue(PyValue o);
PyAPI_FUNC(PyValue) PyNumber_FloatValue(PyValue o);

/* The C number that a value holds, as the PyObject * function of the same
   name without "Value": an inline int or float converts without a box. */
PyAPI_FUNC(long) PyLong_AsLongValue(PyValue o);
PyAPI_FUNC(int) PyLong_AsIntValue(PyValue o);
PyAPI_FUNC(long long) PyLong_AsLongLongValue(PyValue o);
PyAPI_FUNC(Py_ssize_t) PyLong_AsSsize_tValue(PyValue o);
PyAPI_FUNC(Py_ssize_t) PyNumber_AsSsize_tValue(PyValue o, PyObject *exc);
PyAPI_FUNC(double) PyFloat_AsDoubleValue(PyValue o);

PyAPI_FUNC(PyValue) PyObject_GetAttrValue(PyValue o, PyObject *attr_name);
/* PyObject_GetOptionalAttr() on a value: 1 and the attribute in *result, 0
   and PyValue_NULL if there is none, or -1 and PyValue_NULL with an
   exception. */
PyAPI_FUNC(int) PyObject_GetOptionalAttrValue(PyValue o, PyObject *attr_name,
                                              PyValue *result);
/* Deletes the attribute if v is PyValue_NULL. */
PyAPI_FUNC(int) PyObject_SetAttrValue(PyValue o, PyObject *attr_name, PyValue v);
PyAPI_FUNC(PyValue) PyObject_RichCompareValue(PyValue o1, PyValue o2, int opid);
PyAPI_FUNC(int) PyObject_RichCompareBoolValue(PyValue o1, PyValue o2, int opid);
PyAPI_FUNC(int) PyObject_IsTrueValue(PyValue o);
PyAPI_FUNC(Py_hash_t) PyObject_HashValue(PyValue o);

PyAPI_FUNC(PyValue) PyObject_GetItemValue(PyValue o, PyValue key);
PyAPI_FUNC(int) PyObject_SetItemValue(PyValue o, PyValue key, PyValue v);
PyAPI_FUNC(int) PyObject_DelItemValue(PyValue o, PyValue key);
PyAPI_FUNC(int) PySequence_ContainsValue(PyValue seq, PyValue v);
PyAPI_FUNC(PyValue) PyObject_GetIterValue(PyValue o);
/* 1 and the next item in *item, 0 and PyValue_NULL if the iterator is
   exhausted, or -1 and PyValue_NULL with an exception set, as
   PyIter_NextItem(). */
PyAPI_FUNC(int) PyIter_NextValue(PyValue iter, PyValue *item);

PyAPI_FUNC(PyValue) PyObject_StrValue(PyValue o);
PyAPI_FUNC(PyValue) PyObject_ReprValue(PyValue o);
PyAPI_FUNC(PyValue) PyObject_ASCIIValue(PyValue o);
PyAPI_FUNC(PyValue) PyObject_FormatValue(PyValue o, PyObject *format_spec);
