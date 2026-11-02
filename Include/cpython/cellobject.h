/* Cell object interface */

#ifndef Py_LIMITED_API
#ifndef Py_CELLOBJECT_H
#define Py_CELLOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PyObject_HEAD
    /* Strong canonical content, or PyValue_NULL when empty. */
    PyValue ob_ref;
} PyCellObject;

PyAPI_DATA(PyTypeObject) PyCell_Type;

#define PyCell_Check(op) Py_IS_TYPE((op), &PyCell_Type)

PyAPI_FUNC(PyObject *) PyCell_New(PyObject *);
PyAPI_FUNC(PyObject *) PyCell_Get(PyObject *);
PyAPI_FUNC(int) PyCell_Set(PyObject *, PyObject *);
/* NewValue and SetValue borrow their input. GetValue returns an owner. */
PyAPI_FUNC(PyObject *) PyCell_NewValue(PyValue);
PyAPI_FUNC(PyValue) PyCell_GetValue(PyObject *);
PyAPI_FUNC(int) PyCell_SetValue(PyObject *, PyValue);

static inline PyValue
PyCell_GET_VALUE(PyObject *op)
{
    assert(PyCell_Check(op));
    return PyValue_Borrow(_Py_CAST(PyCellObject *, op)->ob_ref);
}
#define PyCell_GET_VALUE(op) PyCell_GET_VALUE(_PyObject_CAST(op))

#ifdef __cplusplus
}
#endif
#endif /* !Py_TUPLEOBJECT_H */
#endif /* Py_LIMITED_API */
