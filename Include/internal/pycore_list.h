#ifndef Py_INTERNAL_LIST_H
#define Py_INTERNAL_LIST_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_value_ops.h"

PyAPI_FUNC(PyObject*) _PyList_Extend(PyListObject *, PyObject *);
PyAPI_FUNC(PyObject) *_PyList_SliceSubscript(PyObject*, PyObject*);
PyAPI_FUNC(PyObject *) _PyList_BinarySlice(PyObject *, PyObject *, PyObject *);
// list[start:stop] = value, with start and stop as PySlice_Unpack() gives
// them for a slice with no step.
extern int _PyList_StoreSlice(PyObject *, Py_ssize_t, Py_ssize_t, PyObject *);
PyAPI_FUNC(PyObject *) _PyList_Concat(PyObject *, PyObject *);
extern void _PyList_DebugMallocStats(FILE *out);
// _PyList_GetItemRef should be used only when the object is known as a list
// because it doesn't raise TypeError when the object is not a list, whereas PyList_GetItemRef does.
Py_VALUE_COMPAT(_PyList_GetValue)
PyAPI_FUNC(PyObject *) _PyList_GetItemRef(PyListObject *, Py_ssize_t i);


#define _PyList_ITEMS(op) _Py_RVALUE(_PyList_CAST(op)->ob_item)

/* A borrow rooted by the list. Retain it (PyValue_DUP()) before a mutation
   can remove its owner. Internal, like every function that returns a
   PyValueBorrow: the operations that read or retain one are internal. */
PyAPI_FUNC(PyValueBorrow) PyList_GET_VALUE(PyObject *op, Py_ssize_t index);
#define PyList_GET_VALUE(op, index) \
    PyList_GET_VALUE(_PyObject_CAST(op), (index))

Py_VALUE_COMPAT(_PyList_AppendTakeValue)
PyAPI_FUNC(int)
_PyList_AppendTakeRefListResize(PyListObject *self, PyObject *newitem);

/* The value forms of PyList_Append() and PyList_Insert(): they borrow their
   input and take a count of their own. Internal, because only the core makes
   a PyValueBorrow; each has a reader macro, so the core passes owners and
   borrows alike. */
PyAPI_FUNC(int) PyList_AppendValue(PyObject *list, PyValueBorrow item);
#define PyList_AppendValue(L, V) (PyList_AppendValue)(L, PyValue_Borrow(V))
PyAPI_FUNC(int) PyList_InsertValue(PyObject *list, Py_ssize_t where,
                                   PyValueBorrow item);
#define PyList_InsertValue(L, I, V) \
    (PyList_InsertValue)(L, I, PyValue_Borrow(V))

/* These consume their input on success and failure. */
Py_VALUE_COMPAT(_PyList_AppendTakeValue)
PyAPI_FUNC(int) _PyList_AppendTakeRef(PyListObject *self, PyObject *newitem);
PyAPI_FUNC(int) _PyList_AppendTakeValue(PyListObject *self, PyValue value);
PyAPI_FUNC(PyValue) _PyList_GetValue(PyListObject *self, Py_ssize_t index);
/* Consumes a strong HeapSafe value on success and failure. NULL is permitted
   only for internal construction/clearing, not as a Python list element. */
PyAPI_FUNC(int) _PyList_SetItemTakeValue(PyListObject *self, Py_ssize_t index,
                                       PyValue value);

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

PyAPI_FUNC(PyObject *)_PyList_FromValueStealOnSuccess(const PyValue *src, Py_ssize_t n);
PyAPI_FUNC(PyObject *)_PyList_AsTupleAndClear(PyListObject *v);

/* Owned value, NULL on exhaustion; these do not box at the object ABI. */
PyAPI_FUNC(_PyValueIndexPair) _PyList_IterValue(PyObject *, Py_ssize_t);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_LIST_H */
