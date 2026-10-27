#ifndef Py_INTERNAL_SETOBJECT_H
#define Py_INTERNAL_SETOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// The next element of a set as its value, and its hash: 1, or 0 at the
// end, or -1 with SystemError if set is not a set.  A caller that needs an
// object makes it with PyValue_Object(), which can fail.  Export for '_abc'
// and '_pickle' shared extensions.
PyAPI_FUNC(int) _PySet_NextValue(
    PyObject *set,
    Py_ssize_t *pos,
    PyValue *key,
    Py_hash_t *hash);

// Export for '_pickle' shared extension
PyAPI_FUNC(int) _PySet_Update(PyObject *set, PyObject *iterable);

// Export for the gdb plugin's (python-gdb.py) benefit
PyAPI_DATA(PyObject *) _PySet_Dummy;

PyAPI_FUNC(int) _PySet_ContainsValue(PySetObject *so, PyValue key);

// Clears the set without acquiring locks. Used by _PyCode_Fini.
extern void _PySet_ClearInternal(PySetObject *so);

PyAPI_FUNC(int) _PySet_AddValue(PySetObject *so, PyValue key);

PyObject *
_PySet_Freeze(PyObject *set);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_SETOBJECT_H
