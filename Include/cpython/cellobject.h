/* Cell object interface */

#ifndef Py_LIMITED_API
#ifndef Py_CELLOBJECT_H
#define Py_CELLOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PyObject_HEAD
    /* Content of the cell, canonical, or PyValue_NULL when empty */
    PyValue ob_ref;
} PyCellObject;

PyAPI_DATA(PyTypeObject) PyCell_Type;

#define PyCell_Check(op) Py_IS_TYPE((op), &PyCell_Type)

PyAPI_FUNC(PyObject *) PyCell_New(PyObject *);
PyAPI_FUNC(PyObject *) PyCell_Get(PyObject *);
PyAPI_FUNC(int) PyCell_Set(PyObject *, PyObject *);

/* The content of the cell as it is: PyValue_NULL when empty. */
static inline PyValue PyCell_GET_VALUE(PyObject *op) {
    PyValue res;
    PyCellObject *cell;
    assert(PyCell_Check(op));
    cell = _Py_CAST(PyCellObject*, op);
    Py_BEGIN_CRITICAL_SECTION(cell);
    res = cell->ob_ref;
    Py_END_CRITICAL_SECTION();
    return res;
}
#define PyCell_GET_VALUE(op) PyCell_GET_VALUE(_PyObject_CAST(op))

static inline void PyCell_SET_VALUE(PyObject *op, PyValue value) {
    PyCellObject *cell;
    assert(PyCell_Check(op));
    cell = _Py_CAST(PyCellObject*, op);
    Py_BEGIN_CRITICAL_SECTION(cell);
    cell->ob_ref = value;
    Py_END_CRITICAL_SECTION();
}
#define PyCell_SET_VALUE(op, value) PyCell_SET_VALUE(_PyObject_CAST(op), (value))

/* The content of the cell as an object: a new box if it is inline, and
   NULL when empty.  NULL means empty only, so it aborts if boxing fails. */
static inline PyObject* PyCell_GET(PyObject *op) {
    return _PyValue_ObjectOrAbort(PyCell_GET_VALUE(op), "PyCell_GET");
}
#define PyCell_GET(op) PyCell_GET(_PyObject_CAST(op))

/* Store value, which may be NULL to empty the cell, canonicalised. */
static inline void PyCell_SET(PyObject *op, PyObject *value) {
    PyCell_SET_VALUE(op, value == NULL ? PyValue_NULL : PyValue_FromObject(value));
}
#define PyCell_SET(op, value) PyCell_SET(_PyObject_CAST(op), (value))

#ifdef __cplusplus
}
#endif
#endif /* !Py_TUPLEOBJECT_H */
#endif /* Py_LIMITED_API */
