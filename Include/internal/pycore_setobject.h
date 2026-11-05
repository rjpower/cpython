#ifndef Py_INTERNAL_SETOBJECT_H
#define Py_INTERNAL_SETOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Owns the returned key; no scalar boxing.
PyAPI_FUNC(int) _PySet_NextValue(
    PyObject *set,
    Py_ssize_t *pos,
    PyValue *key,
    Py_hash_t *hash);

// Borrowed iteration for core code that must not allocate, fail or touch
// reference counts (fault dumps): 1 with a borrowed key view, 0 at the end.
PyAPI_FUNC(int) _PySet_NextBorrowed(PyObject *set, Py_ssize_t *pos, PyValueBorrow *key);

// Export for '_pickle' shared extension
Py_VALUE_COMPAT(_PySet_NextValue)
PyAPI_FUNC(int) _PySet_NextEntryRef(
    PyObject *set,
    Py_ssize_t *pos,
    PyObject **key,
    Py_hash_t *hash);

// Export for '_pickle' shared extension
PyAPI_FUNC(int) _PySet_Update(PyObject *set, PyObject *iterable);

// Export for the gdb plugin's (python-gdb.py) benefit
PyAPI_DATA(PyObject *) _PySet_Dummy;

PyAPI_FUNC(int) _PySet_Contains(PySetObject *so, PyObject *key);

// The value forms of PySet_Contains(), PySet_Add() and PySet_Discard(): they
// borrow the key (AddValue takes a count of its own). Internal, like every
// function that takes or returns a PyValueBorrow; each reader macro lets the
// core pass owners and borrows alike.
PyAPI_FUNC(int) PySet_ContainsValue(PyObject *anyset, PyValueBorrow key);
#define PySet_ContainsValue(S, K) (PySet_ContainsValue)(S, PyValue_Borrow(K))
PyAPI_FUNC(int) PySet_AddValue(PyObject *set, PyValueBorrow key);
#define PySet_AddValue(S, K) (PySet_AddValue)(S, PyValue_Borrow(K))
PyAPI_FUNC(int) PySet_DiscardValue(PyObject *set, PyValueBorrow key);
#define PySet_DiscardValue(S, K) (PySet_DiscardValue)(S, PyValue_Borrow(K))

// Clears the set without acquiring locks. Used by _PyCode_Fini.
extern void _PySet_ClearInternal(PySetObject *so);

Py_VALUE_COMPAT(_PySet_AddValueTakeRef)
PyAPI_FUNC(int) _PySet_AddTakeRef(PySetObject *so, PyObject *key);
PyAPI_FUNC(int) _PySet_AddValueTakeRef(PySetObject *so, PyValue key);
extern PyValue _PySetIter_NextValue(PyObject *iterator);

PyObject *
_PySet_Freeze(PyObject *set);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_SETOBJECT_H
