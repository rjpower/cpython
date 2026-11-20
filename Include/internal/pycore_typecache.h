#ifndef PY_INTERNAL_TYPECACHE_H
#define PY_INTERNAL_TYPECACHE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_value.h"


#define _Py_TYPECACHE_MINSIZE (1 << 3)
#define _Py_TYPECACHE_MAXSIZE (1 << 16)

/* The type dict (or a base's) owns the guarded word; _PyTypeCache_Lookup()
   checks the type version before loading it as a borrow. */
struct type_cache_entry {
    PyObject *name;     // name of the attribute or method, interned string, NULL if the entry is empty
    _PyValueGuarded value;  // canonical value, PyValue_NULL's word for a miss
};

// Per-type attribute lookup cache: speed up attribute and method lookups,
// see _PyTypeCache_Lookup().
struct type_cache {
    uint32_t mask;                                              // mask for indexing into hashtable, i.e. size of hashtable is mask + 1
    uint32_t available;                                         // number of available entries in hashtable
    uint32_t used;                                              // number of used entries in hashtable
    unsigned int version_tag;                                   // initialized from type->tp_version_tag
    struct type_cache_entry hashtable[_Py_TYPECACHE_MINSIZE];   // hashtable entries
};

extern void _PyTypeCache_InitType(PyTypeObject *type);
/* type, name and the canonical semantic result stay independently owned by
   the caller through allocation and cleanup. Insertion never steals them and
   checks the same expected nonzero version again after maintenance callouts. */
extern void _PyTypeCache_Insert(PyTypeObject *type, PyObject *name,
                                PyValueBorrow value, unsigned int expected_version);
#define _PyTypeCache_Insert(T, N, V, E) \
    (_PyTypeCache_Insert)(T, N, PyValue_Borrow(V), E)
/* Return the version tag the entry was found at, or 0 for a miss. A hit stores
   in *out a borrow of the entry's value (PyValue_NULL's word for a cached
   absence). The borrow is valid until Python code can run or a type is
   mutated: see "Version tags and the lookup cache" in typeobject.c. */
PyAPI_FUNC(unsigned int) _PyTypeCache_Lookup(PyTypeObject *type, PyObject *name,
                                             PyValueBorrow *out);
/* Detach publishes an empty table and transfers a private retirement owner.
   Retire consumes it after watcher notification; both are legal during teardown. */
extern struct type_cache *_PyTypeCache_Detach(PyTypeObject *type);
extern void _PyTypeCache_Retire(struct type_cache *cache);
PyAPI_FUNC(void) _PyTypeCache_Invalidate(PyTypeObject *type);

#ifdef __cplusplus
}
#endif
#endif /* PY_INTERNAL_TYPECACHE_H */
