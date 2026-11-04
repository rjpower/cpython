#ifndef Py_INTERNAL_DESCROBJECT_H
#define Py_INTERNAL_DESCROBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef struct {
    PyObject_HEAD
    PyObject *prop_get;
    PyObject *prop_set;
    PyObject *prop_del;
    PyObject *prop_doc;
    PyObject *prop_name;
    int getter_doc;
} propertyobject;

typedef propertyobject _PyPropertyObject;

extern PyTypeObject _PyMethodWrapper_Type;

// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(void *) _PyMember_GetOffset(PyObject *, PyMemberDef *);

/* Value-field descriptors only. Get returns an independent owner; Set borrows
   its input and publishes a strong field before releasing the prior owner. */
extern PyValue _PyMember_GetValue(PyObject *, PyMemberDef *);
extern int _PyMember_SetValue(PyObject *, PyMemberDef *, PyValue);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_DESCROBJECT_H */
