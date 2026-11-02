#ifndef Py_CPYTHON_TUPLEOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct {
    PyObject_VAR_HEAD
    /* Cached hash.  Initially set to -1. */
    Py_hash_t ob_hash;
    /* ob_item contains space for 'ob_size' elements.
       Items must normally not be NULL, except during construction when
       the tuple is not yet visible outside the function that builds it. */
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

/* A borrow rooted by the tuple. Retain it before a callback can remove its
   owner. Generic object-form reads use the owning PyTuple_GetItemRef(). */
static inline PyValue
PyTuple_GET_VALUE(PyObject *op, Py_ssize_t index)
{
    PyTupleObject *tuple = _PyTuple_CAST(op);
    assert(0 <= index && index < Py_SIZE(tuple));
    return PyValue_Borrow(tuple->ob_item[index]);
}
#define PyTuple_GET_VALUE(op, index) \
    PyTuple_GET_VALUE(_PyObject_CAST(op), (index))

/* Function *only* to be used to fill in brand new tuples */
PyAPI_FUNC(void) PyTuple_SET_VALUE(PyObject *op, Py_ssize_t index, PyValue value);
#define PyTuple_SET_VALUE(op, index, value) \
    PyTuple_SET_VALUE(_PyObject_CAST(op), (index), (value))

/* Object-form construction consumes value, publishing before its primitive
   source box is released. */
PyAPI_FUNC(void) PyTuple_SET_ITEM(PyObject *op, Py_ssize_t index, PyObject *value);
#define PyTuple_SET_ITEM(op, index, value) \
    PyTuple_SET_ITEM(_PyObject_CAST(op), (index), _PyObject_CAST(value))

PyAPI_FUNC(PyObject*) PyTuple_FromArray(
    PyObject *const *array,
    Py_ssize_t size);
