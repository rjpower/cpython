#ifndef Py_CPYTHON_TUPLEOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct {
    PyObject_VAR_HEAD
    /* Cached hash.  Initially set to -1. */
    Py_hash_t ob_hash;
    /* ob_item contains space for 'ob_size' elements, each a canonical
       PyValue (Include/pyvalue.h).  Items must normally not be NULL, except
       during construction when the tuple is not yet visible outside the
       function that builds it. */
    PyValue ob_item[1];
} PyTupleObject;

PyAPI_FUNC(int) _PyTuple_Resize(PyObject **, Py_ssize_t);

/* Cast argument to PyTupleObject* type. */
#define _PyTuple_CAST(op) \
    (assert(PyTuple_Check(op)), _Py_CAST(PyTupleObject*, (op)))

// Macros and static inline functions, trading safety for speed

static inline Py_ssize_t PyTuple_GET_SIZE(PyObject *op) {
    PyTupleObject *tuple = _PyTuple_CAST(op);
    return Py_SIZE(tuple);
}
#define PyTuple_GET_SIZE(op) PyTuple_GET_SIZE(_PyObject_CAST(op))

/* The value of item index, which must be in range. */
static inline PyValue
PyTuple_GET_VALUE(PyObject *op, Py_ssize_t index) {
    PyTupleObject *tuple = _PyTuple_CAST(op);
    assert(0 <= index);
    assert(index < Py_SIZE(tuple));
    return tuple->ob_item[index];
}
#define PyTuple_GET_VALUE(op, index) \
    PyTuple_GET_VALUE(_PyObject_CAST(op), (index))

/* The object of item index: a new box if the item is an inline int or
   float (PyValue_Object()), so that two calls may return different
   objects, or NULL with an exception set if boxing fails. */
static inline PyObject *
PyTuple_GET_ITEM(PyObject *op, Py_ssize_t index) {
    return PyValue_Object(PyTuple_GET_VALUE(op, index));
}
#define PyTuple_GET_ITEM(op, index) \
    PyTuple_GET_ITEM(_PyObject_CAST(op), (index))

/* Function *only* to be used to fill in brand new tuples */
static inline void
PyTuple_SET_VALUE(PyObject *op, Py_ssize_t index, PyValue value) {
    PyTupleObject *tuple = _PyTuple_CAST(op);
    assert(0 <= index);
    assert(index < Py_SIZE(tuple));
    tuple->ob_item[index] = value;
}
#define PyTuple_SET_VALUE(op, index, value) \
    PyTuple_SET_VALUE(_PyObject_CAST(op), (index), (value))

/* The same for an object, which is stored canonically: an exact int that
   fits in int32 or an exact float other than NaN is stored inline.  value
   may be NULL. */
static inline void
PyTuple_SET_ITEM(PyObject *op, Py_ssize_t index, PyObject *value) {
    PyTuple_SET_VALUE(op, index,
                      value == NULL ? PyValue_NULL : PyValue_FromObject(value));
}
#define PyTuple_SET_ITEM(op, index, value) \
    PyTuple_SET_ITEM(_PyObject_CAST(op), (index), _PyObject_CAST(value))

PyAPI_FUNC(PyObject*) PyTuple_FromArray(
    PyObject *const *array,
    Py_ssize_t size);
