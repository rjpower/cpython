
/* Named tuple object interface */

#ifndef Py_STRUCTSEQ_H
#define Py_STRUCTSEQ_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct PyStructSequence_Field {
    const char *name;
    const char *doc;
} PyStructSequence_Field;

typedef struct PyStructSequence_Desc {
    const char *name;
    const char *doc;
    PyStructSequence_Field *fields;
    int n_in_sequence;
} PyStructSequence_Desc;

PyAPI_DATA(const char * const) PyStructSequence_UnnamedField;

PyAPI_FUNC(PyTypeObject*) PyStructSequence_NewType(PyStructSequence_Desc *desc);

PyAPI_FUNC(PyObject *) PyStructSequence_New(PyTypeObject* type);

PyAPI_FUNC(void) PyStructSequence_SetItem(PyObject*, Py_ssize_t, PyObject*);
/* Unchecked value reads borrow from the struct sequence; object reads own. */
PyAPI_FUNC(PyValue) PyStructSequence_GET_VALUE(PyObject *, Py_ssize_t);
PyAPI_FUNC(void) PyStructSequence_SET_VALUE(PyObject *, Py_ssize_t, PyValue);
PyAPI_FUNC(PyObject *) PyStructSequence_GetItemRef(PyObject *, Py_ssize_t);

#ifndef Py_LIMITED_API
#  define _Py_CPYTHON_STRUCTSEQ_H
#  include "cpython/structseq.h"
#  undef _Py_CPYTHON_STRUCTSEQ_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_STRUCTSEQ_H */
