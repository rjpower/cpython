#ifndef Py_CPYTHON_DESCROBJECT_H
#  error "this header file must not be included directly"
#endif

/* A slot wrapper's function: self is an object (an inline receiver is boxed
   by the caller) and the result is a value, or PyValue_NULL with an
   exception set. */
typedef PyValue (*wrapperfunc)(PyObject *self, PyObject *args,
                               void *wrapped);

typedef PyValue (*wrapperfunc_kwds)(PyObject *self, PyObject *args,
                                    void *wrapped, PyObject *kwds);

struct wrapperbase {
    const char *name;
    int offset;
    void *function;
    wrapperfunc wrapper;
    const char *doc;
    int flags;
    PyObject *name_strobj;
};

/* Flags for above struct */
#define PyWrapperFlag_KEYWORDS 1 /* wrapper function takes keyword args */

/* Various kinds of descriptor objects */

typedef struct {
    PyObject_HEAD
    PyTypeObject *d_type;
    PyObject *d_name;
    PyObject *d_qualname;
} PyDescrObject;

#define PyDescr_COMMON PyDescrObject d_common

#define PyDescr_TYPE(x) (((PyDescrObject *)(x))->d_type)
#define PyDescr_NAME(x) (((PyDescrObject *)(x))->d_name)

typedef struct {
    PyDescr_COMMON;
    PyMethodDef *d_method;
    vectorcallfunc vectorcall;
} PyMethodDescrObject;

typedef struct {
    PyDescr_COMMON;
    PyMemberDef *d_member;
} PyMemberDescrObject;

typedef struct {
    PyDescr_COMMON;
    PyGetSetDef *d_getset;
} PyGetSetDescrObject;

typedef struct {
    PyDescr_COMMON;
    struct wrapperbase *d_base;
    void *d_wrapped; /* This can be any function pointer */
} PyWrapperDescrObject;

PyAPI_FUNC(PyObject *) PyDescr_NewWrapper(PyTypeObject *,
                                                struct wrapperbase *, void *);
PyAPI_FUNC(int) PyDescr_IsData(PyObject *);

/* PyMember_GetOne() and PyMember_SetOne() on values: a number field reads
   as an inline value where it fits and stores one without a box.  A
   setter's PyValue_NULL deletes. */
PyAPI_FUNC(PyValue) PyMember_GetOneValue(const char *, PyMemberDef *);
PyAPI_FUNC(int) PyMember_SetOneValue(char *, PyMemberDef *, PyValue);
