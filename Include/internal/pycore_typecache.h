#ifndef PY_INTERNAL_TYPECACHE_H
#define PY_INTERNAL_TYPECACHE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_value.h"
#include "pycore_lookup.h"       // _PyLookupPlan


#define _Py_TYPECACHE_MINSIZE (1 << 3)
#define _Py_TYPECACHE_MAXSIZE (1 << 16)

/* An entry is the lookup plan of a name on one shape of the type
   (Include/internal/pycore_lookup.h).  The entry for the type's root shape
   holds the name's MRO result in plan.value, which is what _PyType_Lookup()
   returns; its plan.kind is _Py_LOOKUP_NONE until the name is looked up on
   an object of that shape. */
struct type_cache_entry {
    PyObject *name;     // interned string, NULL if the entry is empty
    _PyShape *shape;    // a shape of the type
    _PyLookupPlan plan;
};

// Per-type cache of lookup plans, keyed by (shape, name): see
// _PyTypeCache_Lookup().  The whole table is dropped when the type changes.
struct type_cache {
    uint32_t mask;                                              // mask for indexing into hashtable, i.e. size of hashtable is mask + 1
    uint32_t available;                                         // number of available entries in hashtable
    uint32_t used;                                              // number of used entries in hashtable
    unsigned int version_tag;                                   // initialized from type->tp_version_tag
    struct type_cache_entry hashtable[_Py_TYPECACHE_MINSIZE];   // hashtable entries
};

struct _PyTypeCacheLookupResult {
    _PyLookupPlan plan;         // a copy of the entry's plan if cache_hit
    int cache_hit;              // 1 if the entry is valid and matches the type's version tag, 0 otherwise
    unsigned int version_tag;   // version tag of the type when the entry was cached
};


extern void _PyTypeCache_InitType(PyTypeObject *type);
// Insert or replace the entry for (SHAPE, NAME).
extern void _PyTypeCache_Insert(PyTypeObject *type, _PyShape *shape,
                                PyObject *name, const _PyLookupPlan *plan);
PyAPI_FUNC(struct _PyTypeCacheLookupResult) _PyTypeCache_Lookup(
    PyTypeObject *type, _PyShape *shape, PyObject *name);
PyAPI_FUNC(void) _PyTypeCache_Invalidate(PyTypeObject *type);
// The bytes of TYPE's cache table, 0 for the shared empty table.
PyAPI_FUNC(size_t) _PyTypeCache_SizeOf(PyTypeObject *type);

#ifdef __cplusplus
}
#endif
#endif /* PY_INTERNAL_TYPECACHE_H */
