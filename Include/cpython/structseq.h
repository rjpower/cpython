#ifndef _Py_CPYTHON_STRUCTSEQ_H
#  error "this header file must not be included directly"
#endif

PyAPI_FUNC(void) PyStructSequence_InitType(PyTypeObject *type,
                                           PyStructSequence_Desc *desc);
PyAPI_FUNC(int) PyStructSequence_InitType2(PyTypeObject *type,
                                           PyStructSequence_Desc *desc);

typedef PyTupleObject PyStructSequence;
#define PyStructSequence_SET_ITEM PyStructSequence_SetItem

/* Unchecked value reads borrow from the struct sequence; object reads own. */
PyAPI_FUNC(PyValueBorrow) PyStructSequence_GET_VALUE(PyObject *, Py_ssize_t);
PyAPI_FUNC(void) PyStructSequence_SET_VALUE(PyObject *, Py_ssize_t, PyValue);
