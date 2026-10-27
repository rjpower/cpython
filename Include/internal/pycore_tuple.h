#ifndef Py_INTERNAL_TUPLE_H
#define Py_INTERNAL_TUPLE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_object.h"        // _PyObject_GC_IS_TRACKED
#include "pycore_structs.h"       // PyValue

extern void _PyTuple_MaybeUntrack(PyObject *);

/* runtime lifecycle */

extern PyStatus _PyTuple_InitGlobalObjects(PyInterpreterState *);


/* other API */

PyAPI_FUNC(void) _PyStolenTuple_Free(PyObject *self);

#define _PyTuple_ITEMS(op) _Py_RVALUE(_PyTuple_CAST(op)->ob_item)

/* The items of tuple op as PyObject *, for a tuple whose items are all
   objects that are never inline, such as keyword names: a PyValue that
   refers to an object is the object's pointer. */
#define _PyTuple_HEAP_ITEMS(op) ((PyObject **)_PyTuple_ITEMS(op))

/* The items of tuple op as an array of PyObject * (_PyValue_ObjectArray()),
   for calls and argument parsing.  NULL with an exception set if boxing
   fails. */
PyAPI_FUNC(PyObject *const *) _PyTuple_Objects(PyObject *op);

PyAPI_FUNC(PyObject *)_PyTuple_FromValues(const struct PyValue *, Py_ssize_t);
PyAPI_FUNC(PyObject *)_PyTuple_FromArraySteal(PyObject *const *, Py_ssize_t);
PyAPI_FUNC(PyObject *) _PyTuple_BinarySlice(PyObject *, PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) _PyTuple_Concat(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) _PyTuple_Repeat(PyObject *self, Py_ssize_t n);

PyAPI_FUNC(PyObject *) _PyTuple_FromPair(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) _PyTuple_FromPairSteal(PyObject *, PyObject *);

typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyTupleObject *it_seq; /* Set to NULL when iterator is exhausted */
} _PyTupleIterObject;

// The next item of a tuple iterator as its value, so that no box is made:
// 1, or 0 at the end.
extern int _PyTupleIter_NextValue(PyObject *it, PyValue *item);

#define _PyTuple_RESET_HASH_CACHE(op)       \
    do {                                    \
        assert(op != NULL);                 \
        _PyTuple_CAST(op)->ob_hash = -1;    \
    } while (0)

/* Below are the official constants from the xxHash specification. Optimizing
   compilers should emit a single "rotate" instruction for the
   _PyTuple_HASH_XXROTATE() expansion. If that doesn't happen for some important
   platform, the macro could be changed to expand to a platform-specific rotate
   spelling instead.
*/
#if SIZEOF_PY_UHASH_T > 4
#define _PyTuple_HASH_XXPRIME_1 ((Py_uhash_t)11400714785074694791ULL)
#define _PyTuple_HASH_XXPRIME_2 ((Py_uhash_t)14029467366897019727ULL)
#define _PyTuple_HASH_XXPRIME_5 ((Py_uhash_t)2870177450012600261ULL)
#define _PyTuple_HASH_XXROTATE(x) ((x << 31) | (x >> 33))  /* Rotate left 31 bits */
#else
#define _PyTuple_HASH_XXPRIME_1 ((Py_uhash_t)2654435761UL)
#define _PyTuple_HASH_XXPRIME_2 ((Py_uhash_t)2246822519UL)
#define _PyTuple_HASH_XXPRIME_5 ((Py_uhash_t)374761393UL)
#define _PyTuple_HASH_XXROTATE(x) ((x << 13) | (x >> 19))  /* Rotate left 13 bits */
#endif
#define _PyTuple_HASH_EMPTY (_PyTuple_HASH_XXPRIME_5 + (_PyTuple_HASH_XXPRIME_5 ^ 3527539UL))

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_TUPLE_H */
