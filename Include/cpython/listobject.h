#ifndef Py_CPYTHON_LISTOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct {
    PyObject_VAR_HEAD
    /* Vector of list elements, each a canonical PyValue (Include/pyvalue.h).
       list[0] is ob_item[0], etc. */
    PyValue *ob_item;

    /* ob_item contains space for 'allocated' elements.  The number
     * currently in use is ob_size.
     * Invariants:
     *     0 <= ob_size <= allocated
     *     len(list) == ob_size
     *     ob_item == NULL implies ob_size == allocated == 0
     * list.sort() temporarily sets allocated to -1 to detect mutations.
     *
     * Items must normally not be NULL, except during construction when
     * the list is not yet visible outside the function that builds it.
     */
    Py_ssize_t allocated;
} PyListObject;

/* Cast argument to PyListObject* type. */
#define _PyList_CAST(op) \
    (assert(PyList_Check(op)), _Py_CAST(PyListObject*, (op)))

// Macros and static inline functions, trading safety for speed

static inline Py_ssize_t PyList_GET_SIZE(PyObject *op) {
    PyListObject *list = _PyList_CAST(op);
    return Py_SIZE(list);
}
#define PyList_GET_SIZE(op) PyList_GET_SIZE(_PyObject_CAST(op))

/* The value of item index, which must be in range. */
static inline PyValue
PyList_GET_VALUE(PyObject *op, Py_ssize_t index) {
    PyListObject *list = _PyList_CAST(op);
    assert(0 <= index);
    assert(index < list->allocated);
    return list->ob_item[index];
}
#define PyList_GET_VALUE(op, index) \
    PyList_GET_VALUE(_PyObject_CAST(op), (index))

/* The object of item index: a new box if the item is an inline int or
   float (PyValue_Object()), so that two calls may return different
   objects.  It cannot report an error, so it aborts if boxing fails. */
static inline PyObject *
PyList_GET_ITEM(PyObject *op, Py_ssize_t index) {
    return _PyValue_ObjectOrAbort(PyList_GET_VALUE(op, index),
                                  "PyList_GET_ITEM");
}
#define PyList_GET_ITEM(op, index) \
    PyList_GET_ITEM(_PyObject_CAST(op), (index))

static inline void
PyList_SET_VALUE(PyObject *op, Py_ssize_t index, PyValue value) {
    PyListObject *list = _PyList_CAST(op);
    assert(0 <= index);
    assert(index < list->allocated);
    list->ob_item[index] = value;
}
#define PyList_SET_VALUE(op, index, value) \
    PyList_SET_VALUE(_PyObject_CAST(op), (index), (value))

/* The same for an object, which is stored canonically: an exact int that
   fits in int32 or an exact float other than NaN is stored inline.  value
   may be NULL. */
static inline void
PyList_SET_ITEM(PyObject *op, Py_ssize_t index, PyObject *value) {
    PyList_SET_VALUE(op, index,
                     value == NULL ? PyValue_NULL : PyValue_FromObject(value));
}
#define PyList_SET_ITEM(op, index, value) \
    PyList_SET_ITEM(_PyObject_CAST(op), (index), _PyObject_CAST(value))

PyAPI_FUNC(int) PyList_Extend(PyObject *self, PyObject *iterable);
PyAPI_FUNC(int) PyList_Clear(PyObject *self);
