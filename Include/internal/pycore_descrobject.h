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
    PyValue prop_get;
    PyValue prop_set;
    PyValue prop_del;
    PyValue prop_doc;
    PyValue prop_name;
    int getter_doc;
} propertyobject;

typedef propertyobject _PyPropertyObject;

extern PyTypeObject _PyMethodWrapper_Type;

/* Positional CALL_METHOD_DESCRIPTOR_FAST after its exact descriptor,
   convention and self-type guards. args includes self; the call borrows the
   initialized window and returns an independent owner. */
extern PyValue _PyMethodDescr_CallFast(PyThreadState *, PyMethodDescrObject *,
                                      const PyValue *args, Py_ssize_t nargs);

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

/* Value getsets. The PyGetSetDef of such an attribute holds the shared thunks
   below, and its closure is a _PyValueGetSet record: only the shared thunks
   and PyDescr_NewGetSet() interpret it. The descriptor's value slots call
   the record's functions directly, without boxing. */
typedef PyValue (*_Py_valuegetter)(PyValueBorrow self, void *closure);
typedef int (*_Py_valuesetter)(PyValueBorrow self, PyValueBorrow value,
                               void *closure);
typedef struct {
    _Py_valuegetter get;
    _Py_valuesetter set;
    void *closure;
} _PyValueGetSet;

// Export for static extension modules that define value getsets
PyAPI_FUNC(PyObject *) _PyGetSet_ValueGet(PyObject *self, void *record);
PyAPI_FUNC(int) _PyGetSet_ValueSet(PyObject *self, PyObject *value,
                                   void *record);

#define _Py_VALUE_GETSET(NAME, VGET, VSET, DOC, CLOSURE) \
    {NAME, _PyGetSet_ValueGet, _PyGetSet_ValueSet, DOC, \
     (void *)&(const _PyValueGetSet){VGET, VSET, CLOSURE}}

// Read-only form: no setter in the PyGetSetDef or in the record.
#define _Py_VALUE_GETTER(NAME, VGET, DOC, CLOSURE) \
    {NAME, _PyGetSet_ValueGet, NULL, DOC, \
     (void *)&(const _PyValueGetSet){VGET, NULL, CLOSURE}}

/* Members as values (the object forms PyMember_GetOne() and PyMember_SetOne()
   are their thunks). Get returns an owner; Set borrows v, which is
   PyValue_NULL for a delete. */
extern PyValue _PyMember_GetOneValue(const char *obj_addr, PyMemberDef *l);
extern int _PyMember_SetOneValue(char *addr, PyMemberDef *l, PyValueBorrow v);
#define _PyMember_SetOneValue(A, M, V) \
    (_PyMember_SetOneValue)(A, M, PyValue_Borrow(V))

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_DESCROBJECT_H */
