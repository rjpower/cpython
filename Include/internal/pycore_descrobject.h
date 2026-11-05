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

/* A slot wrapper descriptor that carries the pair (wrapped, wrapped_value):
   the object function and its value-slot function, which may be NULL
   (value-api-v2a0.md, section 4.5). PyDescr_NewWrapper() passes NULL. */
extern PyObject *_PyDescr_NewWrapperPair(PyTypeObject *type,
                                         struct wrapperbase *base,
                                         void *wrapped, void *wrapped_value);

/* Value-field descriptors only. Get returns an independent owner; Set borrows
   its input and publishes a strong field before releasing the prior owner. */
/* A new method-wrapper of the slot descriptor descr bound to self, which
   it DUPs: the value form of PyWrapper_New() (value-api-m3.md, section
   1.6). self's type must be a subtype of descr's. */
extern PyObject *_PyWrapper_NewValue(PyObject *descr, PyValueBorrow self);
#define _PyWrapper_NewValue(D, SELF) (_PyWrapper_NewValue)(D, PyValue_Borrow(SELF))

extern PyValue _PyMember_GetValue(PyObject *, PyMemberDef *);
extern int _PyMember_SetValue(PyObject *, PyMemberDef *, PyValueBorrow);
#define _PyMember_SetValue(O, M, V) (_PyMember_SetValue)(O, M, PyValue_Borrow(V))

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_DESCROBJECT_H */
