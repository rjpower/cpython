// Implementation of PEP 585: support list[int] etc.
#ifndef Py_GENERICALIASOBJECT_H
#define Py_GENERICALIASOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(PyObject *) Py_GenericAlias(PyObject *, PyObject *);
PyAPI_DATA(PyTypeObject) Py_GenericAliasType;

#ifndef Py_LIMITED_API
/* Py_GenericAlias() in the METH_O convention, for a __class_getitem__
   method: {"__class_getitem__", _PyCFunction_CAST(Py_GenericAliasValue),
   METH_O|METH_CLASS, ...}. */
PyAPI_FUNC(PyValue) Py_GenericAliasValue(PyObject *origin, PyValue args);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_GENERICALIASOBJECT_H */
