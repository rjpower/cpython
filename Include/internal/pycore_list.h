#ifndef Py_INTERNAL_LIST_H
#define Py_INTERNAL_LIST_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


PyAPI_FUNC(PyObject*) _PyList_Extend(PyListObject *, PyObject *);
PyAPI_FUNC(PyObject) *_PyList_SliceSubscript(PyObject*, PyObject*);
PyAPI_FUNC(PyObject *) _PyList_BinarySlice(PyObject *, PyObject *, PyObject *);
// list[start:stop] = value, with start and stop as PySlice_Unpack() gives
// them for a slice with no step.
extern int _PyList_StoreSlice(PyObject *, Py_ssize_t, Py_ssize_t, PyObject *);
PyAPI_FUNC(PyObject *) _PyList_Concat(PyObject *, PyObject *);
// _PyList_GetItemRef should be used only when the object is known as a list
// because it doesn't raise TypeError when the object is not a list, whereas PyList_GetItemRef does.
PyAPI_FUNC(PyObject *) _PyList_GetItemRef(PyListObject *, Py_ssize_t i);



#define _PyList_ITEMS(op) _Py_RVALUE(_PyList_CAST(op)->ob_item)

// v in the list op: 1, 0, or -1 with an exception set.
PyAPI_FUNC(int) _PyList_ContainsValue(PyObject *op, PyValue v);

PyAPI_FUNC(int)
_PyList_AppendValueListResize(PyListObject *self, PyValue newitem);

// Append a canonical value, which the list keeps as it is.
// In free-threaded build: self should be locked by the caller, if it should be thread-safe.
static inline int
_PyList_AppendValue(PyListObject *self, PyValue newitem)
{
    assert(self != NULL && !PyValue_IsNull(newitem));
    assert(PyList_Check(self));
    Py_ssize_t len = Py_SIZE(self);
    Py_ssize_t allocated = self->allocated;
    assert((size_t)len + 1 < PY_SSIZE_T_MAX);
    if (allocated > len) {
        PyList_SET_VALUE(self, len, newitem);
        Py_SET_SIZE(self, len + 1);
        return 0;
    }
    return _PyList_AppendValueListResize(self, newitem);
}

// Append the value of an object: a box of an inline value is not kept.
static inline int
_PyList_AppendTakeRef(PyListObject *self, PyObject *newitem)
{
    assert(newitem != NULL);
    return _PyList_AppendValue(self, PyValue_FromObject(newitem));
}

// Repeat the bytes of a buffer in place
static inline void
_Py_memory_repeat(char* dest, Py_ssize_t len_dest, Py_ssize_t len_src)
{
    assert(len_src > 0);
    Py_ssize_t copied = len_src;
    while (copied < len_dest) {
        Py_ssize_t bytes_to_copy = Py_MIN(copied, len_dest - copied);
        memcpy(dest + copied, dest, (size_t)bytes_to_copy);
        copied += bytes_to_copy;
    }
}

typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyListObject *it_seq; /* Set to NULL when iterator is exhausted */
} _PyListIterObject;

// The next item of a list iterator as its value, so that no box is made:
// 1, or 0 at the end.
extern int _PyListIter_NextValue(PyObject *it, PyValue *item);

struct PyValue;

PyAPI_FUNC(PyObject *)_PyList_FromValues(const struct PyValue *src, Py_ssize_t n);
PyAPI_FUNC(PyObject *)_PyList_AsTupleAndClear(PyListObject *v);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_LIST_H */
