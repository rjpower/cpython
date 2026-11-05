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
/* GetValue returns an owner. NewValue and SetValue, which take a borrowed
   value, are internal (pycore_cell.h), like every function that takes or
   returns a PyValueBorrow. */
PyAPI_FUNC(PyValue) PyCell_GetValue(PyObject *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_TUPLEOBJECT_H */
#endif /* Py_LIMITED_API */
